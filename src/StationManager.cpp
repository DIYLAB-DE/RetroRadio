#include "StationManager.h"
#include "StationsDefault.h"
#include "StringUtils.h"
#include "Logger.h"

#include <esp_partition.h>
#include <mutex>
#include <cstring>

// ===================================================================
// Partitionsname (muss mit partitions.csv übereinstimmen)
// ===================================================================

static constexpr const char* PARTITION_LABEL = "stations";
static constexpr esp_partition_subtype_t PARTITION_SUBTYPE =
    static_cast<esp_partition_subtype_t>(0x80);

// ===================================================================
// Konstruktor
// ===================================================================

/**
 * @brief Initialisiert den StationManager mit leerem Stations-Array
 */
StationManager::StationManager() : m_stationCount(0) {}

// ===================================================================
// CRC32-Berechnung
// ===================================================================

/**
 * @brief Berechnet CRC32 über StationRecord-Daten
 *
 * Verwendet den Standard-CRC32-Algorithmus (Polynom 0xEDB88320). Wird sowohl beim Speichern (Prüfsumme erzeugen) als auch beim Laden (Integrität prüfen) verwendet.
 */
uint32_t StationManager::calculateCRC32(const StationRecord* records, int count) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(records);
    size_t length = count * sizeof(StationRecord);
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }

    return ~crc;
}

// ===================================================================
// Laden aus Flash-Partition
// ===================================================================

/**
 * @brief Lädt Stationen aus der Flash-Partition "stations"
 *
 * Ablauf:
 * 1. Partition suchen
 * 2. Header lesen -> Magic, Version, Count prüfen
 * 3. StationRecords in temporären Buffer lesen
 * 4. CRC32 validieren
 * 5. Daten ins RAM-Array übertragen
 * 6. Bei Fehler -> loadDefaults()
 */
bool StationManager::loadStations() {
    const esp_partition_t* part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, PARTITION_SUBTYPE, PARTITION_LABEL);

    if (!part) {
        LOG_ERROR("Stations-Partition nicht gefunden! Lade Defaults...");
        loadDefaults();
        return true;
    }

    LOG_INFO_F("Stations-Partition gefunden: %s (%u KB)",
               part->label, part->size / 1024);

    // Header lesen
    StationsHeader header;
    esp_err_t err = esp_partition_read(part, 0, &header, sizeof(header));

    if (err != ESP_OK) {
        LOG_ERROR_F("Partition-Read fehlgeschlagen: %s. Lade Defaults...",
                    esp_err_to_name(err));
        loadDefaults();
        return true;
    }

    // Header validieren
    if (header.magic != STATIONS_MAGIC) {
        LOG_WARNING("Stations-Partition leer oder unformatiert (Magic ungültig). Lade Defaults...");
        loadDefaults();
        saveStations();  // Defaults sofort persistieren
        return true;
    }

    if (header.version != STATIONS_FORMAT_VERSION) {
        LOG_WARNING_F("Unbekannte Format-Version %d (erwartet %d). Lade Defaults...",
                      header.version, STATIONS_FORMAT_VERSION);
        loadDefaults();
        saveStations();
        return true;
    }

    if (header.count == 0 || header.count > MAX_STATIONS) {
        LOG_WARNING_F("Ungültige Stationsanzahl: %d. Lade Defaults...", header.count);
        loadDefaults();
        saveStations();
        return true;
    }

    // StationRecords lesen
    const size_t dataSize = header.count * sizeof(StationRecord);

    // PSRAM bevorzugen für den temporären Buffer (~24 KB)
    StationRecord* records = static_cast<StationRecord*>(
        heap_caps_malloc(dataSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (!records) {
        // Fallback auf normalen Heap
        records = static_cast<StationRecord*>(malloc(dataSize));
    }

    if (!records) {
        LOG_ERROR("Kein Speicher für Station-Records! Lade Defaults...");
        loadDefaults();
        return true;
    }

    err = esp_partition_read(part, sizeof(StationsHeader), records, dataSize);

    if (err != ESP_OK) {
        LOG_ERROR_F("Partition-Read der Stationsdaten fehlgeschlagen: %s",
                    esp_err_to_name(err));
        free(records);
        loadDefaults();
        return true;
    }

    // CRC32 prüfen
    uint32_t calculatedCRC = calculateCRC32(records, header.count);

    if (calculatedCRC != header.crc32) {
        LOG_ERROR_F("CRC32-Prüfung fehlgeschlagen! Gespeichert: 0x%08X, Berechnet: 0x%08X. Lade Defaults...",
                    header.crc32, calculatedCRC);
        free(records);
        loadDefaults();
        saveStations();
        return true;
    }

    // Daten ins RAM-Array übertragen
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (int i = 0; i < static_cast<int>(header.count); i++) {
            m_stations[i] = Station();  // Laufzeitfelder zurücksetzen

            stringutils::safeCopy(m_stations[i].shortName, records[i].shortName);
            stringutils::safeCopy(m_stations[i].name,      records[i].name);
            stringutils::safeCopy(m_stations[i].url,       records[i].url);
            m_stations[i].preset = (records[i].preset != 0);
        }

        // Restliche Slots auffüllen (festes 70-Slot-Modell)
        for (int i = header.count; i < MAX_STATIONS; i++) {
            m_stations[i] = Station();
        }
        m_stationCount = MAX_STATIONS;
    }

    free(records);
    LOG_SUCCESS_F("%d Stationen aus Flash-Partition geladen (CRC32 OK)", header.count);
    return true;
}

// ===================================================================
// Speichern in Flash-Partition
// ===================================================================

/**
 * @brief Speichert alle Stationen in die Flash-Partition
 *
 * Ablauf:
 * 1. StationRecords aus RAM-Array in temporären Buffer kopieren
 * 2. CRC32 berechnen
 * 3. Partition löschen (Erase)
 * 4. Header + Records schreiben
 */
bool StationManager::saveStations() {
    const esp_partition_t* part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, PARTITION_SUBTYPE, PARTITION_LABEL);

    if (!part) {
        LOG_ERROR("Stations-Partition nicht gefunden! Speichern nicht möglich.");
        return false;
    }

    int actualCount = 0;

    // Records unter Lock aus RAM kopieren
    const size_t dataSize = MAX_STATIONS * sizeof(StationRecord);

    StationRecord* records = static_cast<StationRecord*>(
        heap_caps_malloc(dataSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (!records) {
        records = static_cast<StationRecord*>(malloc(dataSize));
    }

    if (!records) {
        LOG_ERROR("Kein Speicher für Station-Records beim Speichern!");
        return false;
    }

    memset(records, 0, dataSize);  // Sauberer Ausgangszustand

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        actualCount = m_stationCount;

        for (int i = 0; i < m_stationCount; i++) {
            stringutils::safeCopy(records[i].shortName, m_stations[i].shortName);
            stringutils::safeCopy(records[i].name,      m_stations[i].name);
            stringutils::safeCopy(records[i].url,       m_stations[i].url);
            records[i].preset = m_stations[i].preset ? 1 : 0;
            records[i].padding[0] = 0;
            records[i].padding[1] = 0;
            records[i].padding[2] = 0;
        }
    }
    // Lock freigegeben - Flash-I/O ohne Lock

    // CRC32 berechnen
    uint32_t crc = calculateCRC32(records, actualCount);

    // Header vorbereiten
    StationsHeader header;
    memset(&header, 0, sizeof(header));
    header.magic   = STATIONS_MAGIC;
    header.version = STATIONS_FORMAT_VERSION;
    header.count   = static_cast<uint16_t>(actualCount);
    header.crc32   = crc;

    // Partition löschen (komplette Partition, 4 KB-Sektoren)
    esp_err_t err = esp_partition_erase_range(part, 0, part->size);
    if (err != ESP_OK) {
        LOG_ERROR_F("Partition-Erase fehlgeschlagen: %s", esp_err_to_name(err));
        free(records);
        return false;
    }

    // Header schreiben
    err = esp_partition_write(part, 0, &header, sizeof(header));
    if (err != ESP_OK) {
        LOG_ERROR_F("Header-Write fehlgeschlagen: %s", esp_err_to_name(err));
        free(records);
        return false;
    }

    // Stationsdaten schreiben
    const size_t writeSize = actualCount * sizeof(StationRecord);
    err = esp_partition_write(part, sizeof(StationsHeader), records, writeSize);
    if (err != ESP_OK) {
        LOG_ERROR_F("Stations-Write fehlgeschlagen: %s", esp_err_to_name(err));
        free(records);
        return false;
    }

    free(records);
    LOG_SUCCESS_F("%d Stationen in Flash-Partition gespeichert (CRC32: 0x%08X)",
                  actualCount, crc);
    return true;
}

// ===================================================================
// Defaults laden (PROGMEM)
// ===================================================================

/**
 * @brief Lädt die Standard-Senderliste aus StationsDefault.h ins RAM
 *
 * Wird aufgerufen bei:
 * - Erster Start (Partition leer)
 * - Korrupte Daten (CRC32-Fehler)
 * - Manueller Factory-Reset
 */
void StationManager::loadDefaults() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Array zurücksetzen
    for (int i = 0; i < MAX_STATIONS; i++) {
        m_stations[i] = Station();
    }

    // Defaults aus PROGMEM kopieren
    const int count = (DEFAULT_STATION_COUNT < MAX_STATIONS)
                       ? DEFAULT_STATION_COUNT
                       : MAX_STATIONS;

    for (int i = 0; i < count; i++) {
        stringutils::safeCopy(m_stations[i].shortName, DEFAULT_STATIONS[i].shortName);
        stringutils::safeCopy(m_stations[i].name,      DEFAULT_STATIONS[i].name);
        stringutils::safeCopy(m_stations[i].url,       DEFAULT_STATIONS[i].url);
        m_stations[i].preset = DEFAULT_STATIONS[i].preset;
    }

    m_stationCount = MAX_STATIONS;

    LOG_INFO_F("Default-Senderliste geladen (%d Sender aus PROGMEM)", count);
}

// ===================================================================
// Factory-Reset
// ===================================================================

/**
 * @brief Setzt Senderliste auf Werkseinstellungen zurück und persistiert
 */
bool StationManager::factoryReset() {
    loadDefaults();
    bool saved = saveStations();

    if (saved) {
        LOG_SUCCESS("Senderliste auf Werkseinstellungen zurückgesetzt");
    } else {
        LOG_ERROR("Factory-Reset: Defaults geladen, aber Speichern fehlgeschlagen!");
    }

    return saved;
}

// ===================================================================
// Getter
// ===================================================================

/**
 * @brief Gibt Anzahl geladener Stationen zurück
 */
int StationManager::getStationCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stationCount;
}

/**
 * @brief Gibt Pointer auf Stations-Array zurück
 * @note Lock wird nur kurz gehalten - Pointer danach ungeschützt!
 */
Station* StationManager::getStations() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stations;
}

/**
 * @brief Gibt const Pointer auf Stations-Array zurück
 * @note Lock wird nur kurz gehalten - Pointer danach ungeschützt!
 */
const Station* StationManager::getStations() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stations;
}

/**
 * @brief Gibt URL einer Station zurück
 */
const char* StationManager::getStationUrl(int id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (id < 0 || id >= m_stationCount) return nullptr;
    return m_stations[id].url;
}

/**
 * @brief Gibt Kopie einer einzelnen Station zurück
 */
bool StationManager::getStation(int index, Station& outStation) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (index < 0 || index >= m_stationCount) return false;

    // Sichere Kopie erstellen (Station ist trivial kopierbar)
    outStation = m_stations[index];
    return true;
}

// ===================================================================
// Manipulation
// ===================================================================

/**
 * @brief Löscht alle Stationen
 */
void StationManager::clearStations() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (int i = 0; i < m_stationCount; i++) {
        m_stations[i] = Station();
    }
    m_stationCount = 0;
}

/**
 * @brief Fügt neue Station hinzu
 */
bool StationManager::addStation(const Station& s) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_stationCount >= MAX_STATIONS) return false;

    m_stations[m_stationCount] = s;
    m_stationCount++;

    return true;
}
