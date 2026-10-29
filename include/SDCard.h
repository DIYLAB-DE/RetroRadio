#ifndef SDCARD_H
#define SDCARD_H

#include "FS.h"

/**
 * @brief Gibt Kartentyp, Kartengröße, Gesamtplatz und belegten Platz der SD-Karte auf dem Logger aus.
 */
void getInfos();

/**
 * @brief Listet Dateien und Verzeichnisse rekursiv auf.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param dirname Verzeichnispfad, der aufgelistet werden soll
 * @param levels Rekursionstiefe (0 = nur das angegebene Verzeichnis)
 */
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

/**
 * @brief Erstellt ein Verzeichnis.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path Pfad des Verzeichnisses
 */
void createDir(fs::FS &fs, const char *path);

/**
 * @brief Entfernt ein Verzeichnis.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path Pfad des Verzeichnisses
 */
void removeDir(fs::FS &fs, const char *path);

/**
 * @brief Liest eine Datei und gibt den Inhalt auf dem Logger aus.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path Pfad der Datei
 */
void readFile(fs::FS &fs, const char *path);

/**
 * @brief Schreibt eine Nachricht in eine Datei (überschreibt vorhandenen Inhalt).
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path Pfad der Datei
 * @param message Nachricht, die geschrieben werden soll
 */
void writeFile(fs::FS &fs, const char *path, const char *message);

/**
 * @brief Hängt eine Nachricht an eine bestehende Datei an.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path Pfad der Datei
 * @param message Nachricht, die angehängt werden soll
 */
void appendFile(fs::FS &fs, const char *path, const char *message);

/**
 * @brief Benennt eine Datei um.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path1 Aktueller Pfad der Datei
 * @param path2 Neuer Pfad der Datei
 */
void renameFile(fs::FS &fs, const char *path1, const char *path2);

/**
 * @brief Löscht eine Datei.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path Pfad der Datei
 */
void deleteFile(fs::FS &fs, const char *path);

/**
 * @brief Misst Lese- und Schreib-Durchsatz auf dem angegebenen Dateisystem.
 * @param fs Filesystem-Instanz (z.B. SD, LittleFS)
 * @param path Pfad der Testdatei
 */
void testFileIO(fs::FS &fs, const char *path);

#endif // SDCARD_H
