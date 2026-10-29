#include "SDCard.h"

#include <Arduino.h>

#include "Logger.h"
#include "SD.h"
#include "SPI.h"

// ===================================================================
// Karteninformationen
// ===================================================================

/**
 * @brief Gibt Kartentyp, Kartengröße, Gesamtplatz und belegten Platz aus
 */
void getInfos()
{
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        LOG_WARNING("Keine SD-Karte gefunden");
        return;
    }

    const char *typeName = "UNBEKANNT";
    if (cardType == CARD_MMC)
        typeName = "MMC";
    else if (cardType == CARD_SD)
        typeName = "SDSC";
    else if (cardType == CARD_SDHC)
        typeName = "SDHC";

    LOG_INFO_F("Kartentyp: %s", typeName);

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    LOG_INFO_F("Kartengröße: %llu MB", cardSize);
    LOG_INFO_F("Gesamtplatz: %llu MB", SD.totalBytes() / (1024 * 1024));
    LOG_INFO_F("Belegter Platz: %llu MB", SD.usedBytes() / (1024 * 1024));
}

// ===================================================================
// Verzeichnis-Operationen
// ===================================================================

/**
 * @brief Listet Dateien und Verzeichnisse rekursiv auf
 */
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    LOG_INFO_F("Verzeichnis: %s", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        LOG_ERROR_F("Verzeichnis kann nicht geöffnet werden: %s", dirname);
        return;
    }
    if (!root.isDirectory())
    {
        LOG_ERROR_F("Pfad ist kein Verzeichnis: %s", dirname);
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            LOG_INFO_F("  DIR : %s", file.name());
            if (levels)
                listDir(fs, file.path(), levels - 1);
        }
        else
        {
            LOG_INFO_F("  FILE: %s  SIZE: %u", file.name(), file.size());
        }

        file = root.openNextFile();
    }
}

/**
 * @brief Erstellt ein Verzeichnis
 */
void createDir(fs::FS &fs, const char *path)
{
    LOG_INFO_F("Erstelle Verzeichnis: %s", path);

    if (fs.mkdir(path))
        LOG_SUCCESS("Verzeichnis erstellt");
    else
        LOG_ERROR("Verzeichnis konnte nicht erstellt werden");
}

/**
 * @brief Entfernt ein Verzeichnis
 */
void removeDir(fs::FS &fs, const char *path)
{
    LOG_INFO_F("Entferne Verzeichnis: %s", path);

    if (fs.rmdir(path))
        LOG_SUCCESS("Verzeichnis entfernt");
    else
        LOG_ERROR("Verzeichnis konnte nicht entfernt werden");
}

// ===================================================================
// Datei-Operationen
// ===================================================================

/**
 * @brief Liest eine Datei und gibt den Inhalt auf dem Logger aus
 */
void readFile(fs::FS &fs, const char *path)
{
    LOG_INFO_F("Lese Datei: %s", path);

    File file = fs.open(path);
    if (!file)
    {
        LOG_ERROR_F("Datei konnte nicht zum Lesen geöffnet werden: %s", path);
        return;
    }

    constexpr size_t READ_BUF_SIZE = 512;
    char buf[READ_BUF_SIZE];

    while (file.available())
    {
        size_t toRead = file.available();
        if (toRead > READ_BUF_SIZE - 1)
            toRead = READ_BUF_SIZE - 1;

        size_t bytesRead = file.read(reinterpret_cast<uint8_t *>(buf), toRead);
        buf[bytesRead] = '\0';
        LOG_INFO_F("%s", buf);
    }

    file.close();
}

/**
 * @brief Schreibt eine Nachricht in eine Datei
 */
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    LOG_INFO_F("Schreibe Datei: %s", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        LOG_ERROR_F("Datei konnte nicht zum Schreiben geöffnet werden: %s", path);
        return;
    }

    if (file.print(message))
        LOG_SUCCESS("Datei geschrieben");
    else
        LOG_ERROR("Schreiben fehlgeschlagen");

    file.close();
}

/**
 * @brief Hängt eine Nachricht an eine Datei an
 */
void appendFile(fs::FS &fs, const char *path, const char *message)
{
    LOG_INFO_F("Hänge an Datei an: %s", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        LOG_ERROR_F("Datei konnte nicht zum Anhängen geöffnet werden: %s", path);
        return;
    }

    if (file.print(message))
        LOG_SUCCESS("Nachricht angehängt");
    else
        LOG_ERROR("Anhängen fehlgeschlagen");

    file.close();
}

/**
 * @brief Benennt eine Datei um
 */
void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    LOG_INFO_F("Benenne um: %s -> %s", path1, path2);

    if (fs.rename(path1, path2))
        LOG_SUCCESS("Datei umbenannt");
    else
        LOG_ERROR("Umbenennen fehlgeschlagen");
}

/**
 * @brief Löscht eine Datei
 */
void deleteFile(fs::FS &fs, const char *path)
{
    LOG_INFO_F("Lösche Datei: %s", path);

    if (fs.remove(path))
        LOG_SUCCESS("Datei gelöscht");
    else
        LOG_ERROR("Löschen fehlgeschlagen");
}

// ===================================================================
// Datei-I/O-Benchmark
// ===================================================================

/**
 * @brief Misst Lese- und Schreib-Durchsatz
 */
void testFileIO(fs::FS &fs, const char *path)
{
    constexpr size_t BLOCK_SIZE = 512;
    constexpr size_t WRITE_BLOCKS = 2048;

    static uint8_t buf[BLOCK_SIZE];

    File file = fs.open(path);
    if (file)
    {
        size_t len = file.size();
        size_t total = len;
        uint32_t start = millis();

        while (len)
        {
            size_t toRead = len;
            if (toRead > BLOCK_SIZE)
                toRead = BLOCK_SIZE;

            file.read(buf, toRead);
            len -= toRead;
        }

        uint32_t elapsed = millis() - start;
        LOG_INFO_F("Lesen: %u Bytes in %u ms", total, elapsed);
        file.close();
    }
    else
    {
        LOG_ERROR_F("Datei konnte nicht zum Lesen geöffnet werden: %s", path);
    }

    file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        LOG_ERROR_F("Datei konnte nicht zum Schreiben geöffnet werden: %s", path);
        return;
    }

    uint32_t start = millis();

    for (size_t i = 0; i < WRITE_BLOCKS; i++)
        file.write(buf, BLOCK_SIZE);

    uint32_t elapsed = millis() - start;
    LOG_INFO_F("Schreiben: %u Bytes in %u ms", WRITE_BLOCKS * BLOCK_SIZE, elapsed);
    file.close();
}
